// PR Summary Format Evaluator
// Scores a PR body against the expected release-note format criteria.

const CRITERIA = [
  {
    id: 'h2_heading',
    name: 'H2 Heading',
    description: 'PR body starts with an H2 (##) heading',
    weight: 20,
    test: (body) => /^##\s+\S/m.test(body),
  },
  {
    id: 'bold_summary',
    name: 'Bold Summary',
    description: 'Contains a bold (**...**) summary sentence after the heading',
    weight: 25,
    test: (body) => /^##\s+.+\n+\*\*.+\*\*/m.test(body),
  },
  {
    id: 'changes_section',
    name: 'Changes Section',
    description: 'Contains a "### Changes" subheading',
    weight: 15,
    test: (body) => /^###\s+Changes/mi.test(body),
  },
  {
    id: 'bullet_list',
    name: 'Bullet List',
    description: 'Contains at least one bullet point (- or *)',
    weight: 15,
    test: (body) => /^[\-\*]\s+/m.test(body),
  },
  {
    id: 'bold_category',
    name: 'Bold Category Prefix',
    description: 'At least one bullet has a bold category prefix (e.g., **Updated samples:**)',
    weight: 15,
    test: (body) => /^[\-\*]\s+\*\*.+?\*\*:?/m.test(body),
  },
  {
    id: 'recognized_category',
    name: 'Recognized Category',
    description: 'Uses at least one recognized category (New samples, Updated samples, Updated kits, General)',
    weight: 10,
    test: (body) => {
      const categories = ['new samples', 'updated samples', 'updated kits', 'general'];
      const lower = body.toLowerCase();
      return categories.some((cat) => lower.includes(cat));
    },
  },
];

function evaluate(prBody) {
  if (!prBody || prBody.trim().length === 0) {
    return {
      score: 0,
      maxScore: 100,
      pass: false,
      results: CRITERIA.map((c) => ({
        ...c,
        passed: false,
        earned: 0,
      })),
      feedback: 'PR body is empty. Please add a summary following the release-note format.',
    };
  }

  const results = CRITERIA.map((criterion) => {
    const passed = criterion.test(prBody);
    return {
      id: criterion.id,
      name: criterion.name,
      description: criterion.description,
      weight: criterion.weight,
      passed,
      earned: passed ? criterion.weight : 0,
    };
  });

  const score = results.reduce((sum, r) => sum + r.earned, 0);
  const maxScore = results.reduce((sum, r) => sum + r.weight, 0);
  const pass = score >= 60;

  const missing = results.filter((r) => !r.passed);
  let feedback = '';
  if (pass && missing.length === 0) {
    feedback = 'PR summary follows the expected format.';
  } else if (pass) {
    feedback =
      'PR summary mostly follows the format. Consider adding: ' +
      missing.map((r) => r.name).join(', ') +
      '.';
  } else {
    feedback =
      'PR summary does not meet the format requirements. Missing: ' +
      missing.map((r) => r.name).join(', ') +
      '. See .github/copilot-instructions.md for the expected format.';
  }

  return { score, maxScore, pass, results, feedback };
}

// Main: read PR body from environment or stdin
async function main() {
  const prBody = process.env.PR_BODY || '';
  const result = evaluate(prBody);

  console.log(JSON.stringify(result, null, 2));

  // Output for GitHub Actions
  const githubOutput = process.env.GITHUB_OUTPUT;
  if (githubOutput) {
    const fs = require('fs');
    fs.appendFileSync(githubOutput, `score=${result.score}\n`);
    fs.appendFileSync(githubOutput, `max_score=${result.maxScore}\n`);
    fs.appendFileSync(githubOutput, `pass=${result.pass}\n`);
    fs.appendFileSync(githubOutput, `feedback=${result.feedback}\n`);
  }

  process.exit(result.pass ? 0 : 1);
}

module.exports = { evaluate, CRITERIA };

if (require.main === module) {
  main();
}
